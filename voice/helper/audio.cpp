// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "audio.h"

#include "common.h"

#include <pulse/error.h>
#include <pulse/simple.h>

#include <cstdlib>
#include <thread>

namespace voice {

Audio::~Audio()
{
    if (m_pulse)
        pa_simple_free(m_pulse);
    if (m_file)
        std::fclose(m_file);
}

bool Audio::open(std::string &error)
{
    if (const char *wav = std::getenv("PARROTFISH_VOICE_WAV")) {
        m_file = std::fopen(wav, "rb");
        if (!m_file || std::fseek(m_file, 44, SEEK_SET) != 0) {
            error = std::string("cannot read ") + wav;
            return false;
        }
        m_next = now();
        return true;
    }
    pa_sample_spec spec;
    spec.format = PA_SAMPLE_FLOAT32LE;
    spec.rate = Rate;
    spec.channels = 1;
    pa_buffer_attr attr;
    attr.maxlength = static_cast<uint32_t>(-1);
    attr.tlength = attr.prebuf = attr.minreq = static_cast<uint32_t>(-1);
    attr.fragsize = Window * sizeof(float);
    int code = 0;
    m_pulse = pa_simple_new(nullptr, "Parrotfish", PA_STREAM_RECORD, nullptr, "Voice input",
                            &spec, nullptr, &attr, &code);
    if (!m_pulse)
        error = pa_strerror(code);
    return m_pulse != nullptr;
}

bool Audio::read(std::vector<float> &chunk, std::string &error)
{
    if (m_file) {
        m_next += double(chunk.size()) / Rate;
        const double wait = m_next - now();
        if (wait > 0)
            std::this_thread::sleep_for(std::chrono::duration<double>(wait));
        std::vector<int16_t> raw(chunk.size(), 0);
        const size_t got = std::fread(raw.data(), sizeof(int16_t), raw.size(), m_file);
        for (size_t i = 0; i < chunk.size(); ++i)
            chunk[i] = i < got ? raw[i] / 32768.0f : 0.0f;
        return true;
    }
    int code = 0;
    if (pa_simple_read(m_pulse, chunk.data(), chunk.size() * sizeof(float), &code) < 0) {
        error = pa_strerror(code);
        return false;
    }
    return true;
}

} // namespace voice
