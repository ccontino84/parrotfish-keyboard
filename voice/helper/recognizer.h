// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include <string>
#include <utility>
#include <vector>

struct SherpaOnnxOfflineRecognizer;

namespace voice {

// Parakeet through sherpa-onnx's C API: audio in, tokens with times out.
class Recognizer
{
public:
    ~Recognizer();

    // `dir` holds encoder.int8.onnx, decoder.int8.onnx, joiner.int8.onnx
    // and tokens.txt.
    bool load(const std::string &dir, int threads);

    struct Token {
        std::string text;
        float time;   // seconds from the start of the decoded audio
    };

    std::vector<Token> tokens(const float *samples, int n);
    // Longer audio in pieces of ChunkSeconds.
    std::string decode(const std::vector<float> &audio);

    // The text of the tokens from `from` to `to` seconds.
    static std::string join(const std::vector<Token> &tokens, float from = -1e9f, float to = 1e9f);
    // Splits tokens at `left` and `split` seconds into the text between
    // them and the text after (what comes before `left` is dropped):
    // punctuation comes a little after its word, so at a cut it stays with
    // the word before.
    static std::pair<std::string, std::string> split(const std::vector<Token> &tokens, float left, float split);
    static std::string append(const std::string &text, const std::string &part);
    // The decode time, for the log.
    static void logTime(double since, size_t samples);

private:
    static bool punctuation(const std::string &token);

    const SherpaOnnxOfflineRecognizer *m_recognizer = nullptr;
};

} // namespace voice
