// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "vad.h"

#include <sherpa-onnx/c-api/c-api.h>

#include <cmath>
#include <cstring>

namespace voice {

Vad::~Vad()
{
    if (m_vad)
        SherpaOnnxDestroyVoiceActivityDetector(m_vad);
}

bool Vad::open(const std::string &model)
{
    SherpaOnnxVadModelConfig config;
    std::memset(&config, 0, sizeof(config));
    config.silero_vad.model = model.c_str();
    config.silero_vad.threshold = 0.5f;
    config.silero_vad.min_silence_duration = 0.5f;
    config.silero_vad.min_speech_duration = 0.25f;
    config.silero_vad.window_size = Window;
    config.silero_vad.max_speech_duration = 30.0f;
    config.sample_rate = Rate;
    config.num_threads = 1;
    config.provider = "cpu";
    m_vad = SherpaOnnxCreateVoiceActivityDetector(&config, 60.0f);
    return m_vad != nullptr;
}

std::vector<Segment> Vad::feed(const float *chunk)
{
    float peak = 0.0f;
    for (int i = 0; i < Window; ++i)
        peak = std::max(peak, std::fabs(chunk[i]));
    m_peak = std::max(peak, m_peak * 0.995f);
    const float gain = std::min(20.0f, std::max(1.0f, 0.25f / std::max(m_peak, 1e-4f)));
    for (int i = 0; i < Window; ++i)
        m_scaled[i] = std::max(-1.0f, std::min(1.0f, chunk[i] * gain));
    SherpaOnnxVoiceActivityDetectorAcceptWaveform(m_vad, m_scaled.data(), Window);
    return take();
}

std::vector<Segment> Vad::finish()
{
    const std::vector<float> silence(Window, 0.0f);
    for (int i = 0; i < Rate * 6 / 10 / Window; ++i)
        SherpaOnnxVoiceActivityDetectorAcceptWaveform(m_vad, silence.data(), Window);
    SherpaOnnxVoiceActivityDetectorFlush(m_vad);
    return take();
}

std::vector<Segment> Vad::take()
{
    std::vector<Segment> out;
    while (!SherpaOnnxVoiceActivityDetectorEmpty(m_vad)) {
        const SherpaOnnxSpeechSegment *s = SherpaOnnxVoiceActivityDetectorFront(m_vad);
        const Segment segment{s->start, s->start + s->n};
        SherpaOnnxDestroySpeechSegment(s);
        SherpaOnnxVoiceActivityDetectorPop(m_vad);
        if (segment.end - segment.start >= Rate / 10)
            out.push_back(segment);
    }
    return out;
}

} // namespace voice
