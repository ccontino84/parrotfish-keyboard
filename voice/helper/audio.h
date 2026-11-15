// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include <cstdio>
#include <string>
#include <vector>

struct pa_simple;

namespace voice {

// The microphone through PulseAudio (16 kHz mono float), or with
// PARROTFISH_VOICE_WAV (tests) a 16 kHz mono s16 WAV file read in real
// time, then silence.
class Audio
{
public:
    ~Audio();

    bool open(std::string &error);
    bool read(std::vector<float> &chunk, std::string &error);

private:
    pa_simple *m_pulse = nullptr;
    std::FILE *m_file = nullptr;
    double m_next = 0;
};

} // namespace voice
