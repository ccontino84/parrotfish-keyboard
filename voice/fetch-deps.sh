#!/bin/sh
# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Downloads the voice package's prebuilt dependencies into voice/deps:
# sherpa-onnx's aarch64 C API (with its onnxruntime) and Silero VAD, pinned
# by version and SHA-256. Needed before an aarch64 RPM build.
#
#   voice/fetch-deps.sh
set -eu

sherpa=2.0.1
lib_archive=sherpa-onnx-v$sherpa-linux-aarch64-shared-cpu-lib
lib_sha=15bd737eae467da32ec97e3740a329a535fd21e33caca4f327b0081451d11533
header_sha=5698f38ba09ad2bdf085c56a6cbbd7741acf15e25f4ce5c7097b9702cadf437f
vad_sha=9e2449e1087496d8d4caba907f23e0bd3f78d91fa552479bb9c23ac09cbb1fd6

deps=$(cd "$(dirname "$0")" && pwd)/deps
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

fetch() { # url sha256 file
    curl -fsSL --retry 3 -o "$3" "$1"
    echo "$2  $3" | sha256sum -c --quiet -
}

fetch "https://github.com/k2-fsa/sherpa-onnx/releases/download/v$sherpa/$lib_archive.tar.bz2" \
    "$lib_sha" "$tmp/lib.tar.bz2"
fetch "https://raw.githubusercontent.com/k2-fsa/sherpa-onnx/v$sherpa/sherpa-onnx/c-api/c-api.h" \
    "$header_sha" "$tmp/c-api.h"
fetch "https://github.com/k2-fsa/sherpa-onnx/releases/download/asr-models/silero_vad.onnx" \
    "$vad_sha" "$tmp/silero_vad.onnx"

tar -xjf "$tmp/lib.tar.bz2" -C "$tmp"
mkdir -p "$deps/sherpa-onnx/lib" "$deps/sherpa-onnx/include/sherpa-onnx/c-api" "$deps/models"
cp "$tmp/$lib_archive/lib/libsherpa-onnx-c-api.so" "$tmp/$lib_archive/lib/libonnxruntime.so" \
    "$deps/sherpa-onnx/lib/"
cp "$tmp/c-api.h" "$deps/sherpa-onnx/include/sherpa-onnx/c-api/"
cp "$tmp/silero_vad.onnx" "$deps/models/"
echo "voice dependencies in $deps"
