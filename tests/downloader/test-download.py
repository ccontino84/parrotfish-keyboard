#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LGPL-2.1-or-later
"""parrotfish-download language CODE against a local HTTP server: the
download, its checks, resuming, and that a failed download leaves the
installed dictionary as it was.

usage: test-download.py PARROTFISH_DOWNLOAD
"""
import gzip
import hashlib
import http.server
import json
import os
import subprocess
import sys
import tempfile
import threading

PROGRAM = sys.argv[1]
failures = 0


def check(ok, what, got=""):
    global failures
    if not ok:
        failures += 1
        print(f"FAIL {what}" + (f": {got}" if got else ""))


class Server(http.server.BaseHTTPRequestHandler):
    files = {}         # path: bytes
    ranges = True      # answers Range with 206
    requests = []

    def do_GET(self):
        Server.requests.append((self.path, self.headers.get("Range")))
        body = Server.files.get(self.path)
        if body is None:
            self.send_error(404)
            return
        start = 0
        rng = self.headers.get("Range")
        if rng and Server.ranges:
            start = int(rng.split("=")[1].rstrip("-"))
            self.send_response(206)
            self.send_header("Content-Range", f"bytes {start}-{len(body) - 1}/{len(body)}")
        else:
            self.send_response(200)
        self.send_header("Content-Length", str(len(body) - start))
        self.end_headers()
        self.wfile.write(body[start:])

    def log_message(self, *args):
        pass


server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), Server)
threading.Thread(target=server.serve_forever, daemon=True).start()
base = f"http://127.0.0.1:{server.server_port}"

tmp = tempfile.mkdtemp()
data = os.path.join(tmp, "data")
home = os.path.join(tmp, "home")
dicts = os.path.join(home, ".local/share/parrotfish-keyboard/dictionaries")
os.makedirs(data)
env = dict(os.environ, PARROTFISH_DATA_DIR=data, HOME=home)


def sha(b):
    return hashlib.sha256(b).hexdigest()


def publish(code, content, path=None, gz_hash=None, pfd_hash=None, size=None):
    """Serves CODE's dictionary and lists it in languages.json."""
    gz = gzip.compress(content, mtime=0)
    path = path or f"/languages-1/{code}.pfd.gz"
    Server.files[path] = gz
    catalog = {}
    if os.path.exists(os.path.join(data, "languages.json")):
        catalog = json.load(open(os.path.join(data, "languages.json")))
    catalog[code] = {"name": code, "download": {
        "url": base + path, "size": size or len(gz), "sha256": gz_hash or sha(gz),
        "pfdSize": len(content), "pfdSha256": pfd_hash or sha(content)}}
    json.dump(catalog, open(os.path.join(data, "languages.json"), "w"))
    return gz


def run(code):
    p = subprocess.run([PROGRAM, "language", code], env=env, capture_output=True, text=True, timeout=60)
    return p.returncode, p.stdout.strip().splitlines()


def installed(code):
    pfd = os.path.join(dicts, code + ".pfd")
    if not os.path.exists(pfd) or not os.path.exists(pfd + ".sha256"):
        return None
    return open(pfd, "rb").read(), open(pfd + ".sha256").read().strip()


first = b"the first dictionary " * 5000
second = b"the second dictionary " * 6000

# A download.
publish("xx", first)
code, out = run("xx")
check(code == 0 and out[-1] == "done", "download", " | ".join(out))
check(installed("xx") == (first, sha(first)), "the dictionary and its record")
check(sorted(os.listdir(dicts)) == ["xx.pfd", "xx.pfd.sha256"], "nothing else left", str(os.listdir(dicts)))
check(any(line.startswith("progress ") for line in out), "progress lines")

# An update replaces it.
publish("xx", second)
code, out = run("xx")
check(code == 0 and installed("xx") == (second, sha(second)), "update", " | ".join(out))

# A damaged download, or a dictionary not as listed: the old one stays.
publish("xx", first, gz_hash="0" * 64)
code, out = run("xx")
check(code == 1 and out[-1] == "error checksum xx.pfd.gz", "wrong .gz hash", " | ".join(out))
check(installed("xx") == (second, sha(second)), "old one kept after a damaged download")
publish("xx", first, pfd_hash="1" * 64)
code, out = run("xx")
check(code == 1 and out[-1] == "error checksum xx.pfd", "wrong dictionary hash", " | ".join(out))
check(installed("xx") == (second, sha(second)), "old one kept after a wrong dictionary")
check(sorted(os.listdir(dicts)) == ["xx.pfd", "xx.pfd.sha256"], "failed files removed", str(os.listdir(dicts)))

# Resumed from a partial file: only the rest is asked for.
gz = publish("yy", first)
with open(os.path.join(dicts, "yy.pfd.gz.part"), "wb") as f:
    f.write(gz[:100])
Server.requests.clear()
code, out = run("yy")
check(code == 0 and installed("yy") == (first, sha(first)), "resumed", " | ".join(out))
check(Server.requests == [("/languages-1/yy.pfd.gz", "bytes=100-")], "asked for the rest", str(Server.requests))

# A server that ignores Range: starts over.
Server.ranges = False
gz = publish("zz", first)
with open(os.path.join(dicts, "zz.pfd.gz.part"), "wb") as f:
    f.write(gz[:100])
code, out = run("zz")
check(code == 0 and installed("zz") == (first, sha(first)), "no partial answer: starts over", " | ".join(out))
Server.ranges = True

# Not found, too short, not listed.
publish("nf", first, path="/elsewhere/nf.pfd.gz")
del Server.files["/elsewhere/nf.pfd.gz"]
code, out = run("nf")
check(code == 1 and out[-1].startswith("error network"), "not found", " | ".join(out))
gz = publish("sh", first)
publish("sh", first, size=len(gz) + 10, gz_hash=sha(gz))
code, out = run("sh")
check(code == 1 and out[-1] == "error network incomplete sh.pfd.gz", "too short", " | ".join(out))
code, out = run("qq")
check(code == 1 and out == ["error unavailable"], "not in languages.json", " | ".join(out))

server.shutdown()
print(f"{failures} failures" if failures else "all passed")
sys.exit(1 if failures else 0)
