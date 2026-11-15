// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include "common.h"

#include <string>
#include <vector>

struct SherpaOnnxVoiceActivityDetector;

namespace voice {

// Silero VAD through sherpa-onnx: the recording's audio in, the phrases
// (speech between pauses of 0.5 s) out. It hears a levelled copy, so that
// quiet speakers aren't missed.
class Vad
{
public:
    ~Vad();

    bool open(const std::string &model);
    // A chunk of Window samples; the phrases that ended with it.
    std::vector<Segment> feed(const float *chunk);
    // Ends the last phrase as a pause would.
    std::vector<Segment> finish();

private:
    std::vector<Segment> take();

    const SherpaOnnxVoiceActivityDetector *m_vad = nullptr;
    float m_peak = 0.0f;
    std::vector<float> m_scaled = std::vector<float>(Window);
};

} // namespace voice
