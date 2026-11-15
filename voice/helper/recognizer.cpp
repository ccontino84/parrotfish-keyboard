// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "recognizer.h"

#include "common.h"

#include <sherpa-onnx/c-api/c-api.h>

#include <cstring>

namespace voice {

Recognizer::~Recognizer()
{
    if (m_recognizer)
        SherpaOnnxDestroyOfflineRecognizer(m_recognizer);
}

bool Recognizer::load(const std::string &dir, int threads)
{
    SherpaOnnxOfflineRecognizerConfig config;
    std::memset(&config, 0, sizeof(config));
    const std::string encoder = dir + "/encoder.int8.onnx";
    const std::string decoder = dir + "/decoder.int8.onnx";
    const std::string joiner = dir + "/joiner.int8.onnx";
    const std::string tokens = dir + "/tokens.txt";
    config.feat_config.sample_rate = Rate;
    config.feat_config.feature_dim = 80;
    config.model_config.transducer.encoder = encoder.c_str();
    config.model_config.transducer.decoder = decoder.c_str();
    config.model_config.transducer.joiner = joiner.c_str();
    config.model_config.tokens = tokens.c_str();
    config.model_config.num_threads = threads;
    config.model_config.provider = "cpu";
    config.model_config.model_type = "nemo_transducer";
    config.decoding_method = "greedy_search";
    m_recognizer = SherpaOnnxCreateOfflineRecognizer(&config);
    return m_recognizer != nullptr;
}

std::vector<Recognizer::Token> Recognizer::tokens(const float *samples, int n)
{
    std::vector<Token> out;
    if (!m_recognizer || n <= 0)
        return out;
    const SherpaOnnxOfflineStream *stream = SherpaOnnxCreateOfflineStream(m_recognizer);
    SherpaOnnxAcceptWaveformOffline(stream, Rate, samples, n);
    SherpaOnnxDecodeOfflineStream(m_recognizer, stream);
    if (const SherpaOnnxOfflineRecognizerResult *r = SherpaOnnxGetOfflineStreamResult(stream)) {
        for (int i = 0; i < r->count; ++i)
            out.push_back({r->tokens_arr[i], r->timestamps ? r->timestamps[i] : 0.0f});
        SherpaOnnxDestroyOfflineRecognizerResult(r);
    }
    SherpaOnnxDestroyOfflineStream(stream);
    return out;
}

std::string Recognizer::decode(const std::vector<float> &audio)
{
    std::string text;
    for (size_t at = 0; at < audio.size(); at += ChunkSeconds * Rate) {
        const size_t n = std::min<size_t>(audio.size() - at, ChunkSeconds * Rate);
        text = append(text, join(tokens(audio.data() + at, static_cast<int>(n))));
    }
    return text;
}

std::string Recognizer::join(const std::vector<Token> &tokens, float from, float to)
{
    std::string s;
    for (auto &t : tokens)
        if (t.time >= from && t.time < to)
            s += t.text;
    const std::string mark = "\xe2\x96\x81";   // SentencePiece's word start
    for (size_t p; (p = s.find(mark)) != std::string::npos;)
        s.replace(p, mark.size(), " ");
    const auto first = s.find_first_not_of(' ');
    const auto last = s.find_last_not_of(' ');
    return first == std::string::npos ? std::string() : s.substr(first, last - first + 1);
}

std::pair<std::string, std::string> Recognizer::split(const std::vector<Token> &tokens, float left, float split)
{
    size_t i = 0, j;
    while (i < tokens.size() && tokens[i].time < left)
        ++i;
    while (i < tokens.size() && punctuation(tokens[i].text))
        ++i;
    j = i;
    while (j < tokens.size() && tokens[j].time < split)
        ++j;
    while (j < tokens.size() && punctuation(tokens[j].text))
        ++j;
    const std::vector<Token> a(tokens.begin() + i, tokens.begin() + j), b(tokens.begin() + j, tokens.end());
    return {join(a), join(b)};
}

std::string Recognizer::append(const std::string &text, const std::string &part)
{
    return text.empty() || part.empty() ? text + part : text + " " + part;
}

void Recognizer::logTime(double since, size_t samples)
{
    log("decoded " + std::to_string(samples / double(Rate)).substr(0, 5) + " s in "
        + std::to_string(now() - since).substr(0, 5) + " s");
}

bool Recognizer::punctuation(const std::string &token)
{
    bool any = false;
    for (char c : token) {
        if (c == ' ')
            continue;
        if (!std::strchr(",.;:!?", c))
            return false;
        any = true;
    }
    return any;
}

} // namespace voice
