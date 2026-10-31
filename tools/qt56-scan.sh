#!/bin/bash
# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LicenseRef-TBD
#
# The phone runs Qt 5.6, a PC usually Qt 5.15: lint and the PC tests accept
# code that Qt 5.6 rejects. Flags the known traps in the repo's QML and JS:
# keywords after a dot in QML files (Qt 5.6 lexes "as" and "import" as
# keywords there: k.as broke alpha26) and newer JavaScript (let, const,
# arrow functions, template strings, spread).
# usage: tools/qt56-scan.sh [REPO]   (exit 1 on a hit)
REPO=${1:-$(dirname "$(cd "$(dirname "$0")" && pwd)")}
cd "$REPO" || exit 2
hits=$(grep -n -E '\.(as|import|pragma)\b|=>|\blet |\bconst |`|\.\.\.[A-Za-z_]' \
        $(git ls-files '*.qml' '*.js') \
    | grep -v -E '^[^:]+:[0-9]+:\s*(//|\*|/\*|\.pragma|import )' \
    | grep -v -E '"`"')
if [ -n "$hits" ]; then echo "$hits"; exit 1; fi
echo "no Qt 5.6 traps found"
