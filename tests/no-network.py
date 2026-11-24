#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LGPL-2.1-or-later
"""The keyboard itself never connects to the internet (README, Safety):
its plugin calls no network function and its QML makes no web requests.
Only parrotfish-download, started by the settings, uses the network.

usage: no-network.py PLUGIN_SO SOURCE_DIR
"""
import os
import re
import subprocess
import sys

plugin, source = sys.argv[1], sys.argv[2]
problems = []
# The QML engine links Qt's network library anyway: what counts is that our
# code calls none of it.
symbols = subprocess.run(["nm", "-DC", "--undefined-only", plugin], capture_output=True, text=True,
                         check=True).stdout
for line in symbols.splitlines():
    if re.search(r"\bQ(Network|Tcp|Udp|Ssl|AbstractSocket|HostAddress|LocalSocket)\w*::|curl_|soup_", line):
        problems.append(f"{os.path.basename(plugin)} calls {line.split(maxsplit=1)[-1]}")
for folder in ("handler", "layouts", "plugin"):
    for root, _, files in os.walk(os.path.join(source, folder)):
        for name in files:
            if not name.endswith((".qml", ".js")):
                continue
            path = os.path.join(root, name)
            for n, line in enumerate(open(path, encoding="utf-8"), 1):
                code = line.split("//")[0] if "://" not in line.split("//")[0] else line
                if re.search(r"XMLHttpRequest|\bhttps?://", code) and not line.lstrip().startswith("//"):
                    problems.append(f"{os.path.relpath(path, source)}:{n}: {line.strip()}")
for p in problems:
    print("FAIL", p)
sys.exit(1 if problems else 0)
