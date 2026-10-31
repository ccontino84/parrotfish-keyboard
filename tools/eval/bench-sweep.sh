#!/bin/sh
# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LicenseRef-TBD
#
# Runs sentence-bench once per value of an engine parameter and prints its
# summary lines side by side: the trade-off between typos fixed, wrong fixes,
# words changed and the word error rate.
#
# usage: tools/eval/bench-sweep.sh PARAM "VALUE..." SENTENCE-BENCH-ARGS...
#   tools/eval/bench-sweep.sh normalConfidence "0.3 0.5 0.7 0.85" \
#       -l data/layouts/phone-qwerty.layout -t tests/data/sentences-en.txt -m en \
#       -d build/data/en.pfd -d build/data/it.pfd -d build/data/de.pfd \
#       -X -N 75 -o build/data/oov-en.txt
# BENCH=path overrides build/tests/sentence-bench.
set -e
param=$1
values=$2
shift 2
bench=${BENCH:-build/tests/sentence-bench}
for v in $values; do
    printf '%s=%-6s ' "$param" "$v"
    "$bench" "$@" -P "$param=$v" | sed -n 's/^summary: //p'
done
