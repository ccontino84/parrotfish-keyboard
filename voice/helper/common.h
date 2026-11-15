// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <string>

namespace voice {

constexpr int Rate = 16000;
constexpr int Window = 512;          // Silero VAD's window at 16 kHz
constexpr int ChunkSeconds = 25;     // the longest audio decoded at once
constexpr int ContextSeconds = 3;    // the audio decoded before a pair

struct Segment {
    int64_t start;   // samples from the start of the recording
    int64_t end;
};

inline double now()
{
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

// One line to the plugin (stdout).
inline void emit(const std::string &line)
{
    static std::mutex mutex;
    std::lock_guard<std::mutex> lock(mutex);
    std::string clean = line;
    std::replace(clean.begin(), clean.end(), '\n', ' ');
    std::fputs(clean.c_str(), stdout);
    std::fputc('\n', stdout);
    std::fflush(stdout);
}

// One line to the log (stderr, ~/.cache/parrotfish-keyboard/voice.log).
inline void log(const std::string &line)
{
    std::fprintf(stderr, "%s\n", line.c_str());
    std::fflush(stderr);
}

} // namespace voice
