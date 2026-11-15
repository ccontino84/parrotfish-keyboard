// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// parrotfish-voice MODEL_DIR: offline speech recognition for the keyboard,
// started by the plugin (VoiceInput) and driven through stdin/stdout, one
// line each.
//   in:  start | continue | stop | cancel | quit
//   out: loading | ready | listening | level <0-1> | fixed <phrase> <text>
//        | open <phrase> <text> | final <text> | idle | error <text>
// It records while listening and cuts the audio into phrases at pauses. At
// each pause it decodes the newest phrase together with the one before
// (Phrases): that one's text won't change any more (fixed), the newest one's
// may (open, replacing the previous open text). Phrases are numbered from 0
// in each recording. On stop, final is all of it. continue starts like
// start, but decodes the first phrase after the end of the previous
// recording's speech, so that a short phrase keeps its language. It quits
// after some idle minutes, so that the model doesn't stay in memory, and
// can't use the network (sandbox.cpp).

#include "audio.h"
#include "common.h"
#include "livetext.h"
#include "phrases.h"
#include "recognizer.h"
#include "sandbox.h"
#include "vad.h"
#include "worker.h"

#include <sched.h>

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <memory>

#ifndef PARROTFISH_VOICE_DATA
#define PARROTFISH_VOICE_DATA "/usr/share/parrotfish-keyboard-voice"
#endif

using namespace voice;

namespace {

constexpr int MaxSeconds = 120;   // a recording stops by itself after this
constexpr int IdleExitSeconds = 300;

// The CPUs with the highest maximum frequency: the big cores.
std::vector<int> bigCores()
{
    std::vector<std::pair<int, long>> cpus;
    for (int i = 0; i < 64; ++i) {
        std::ifstream f("/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/cpuinfo_max_freq");
        long freq = 0;
        if (f >> freq)
            cpus.push_back({i, freq});
    }
    long top = 0;
    for (auto &c : cpus)
        top = std::max(top, c.second);
    std::vector<int> big;
    for (auto &c : cpus)
        if (c.second == top)
            big.push_back(c.first);
    return big;
}

std::string phraseLine(const char *word, long phrase, const std::string &text)
{
    return std::string(word) + " " + std::to_string(phrase) + " " + text;
}

} // namespace

int main(int argc, char **argv)
{
    if (argc < 2) {
        std::fprintf(stderr, "usage: %s MODEL_DIR\n", argv[0]);
        return 2;
    }
    // First, before any thread: the helper never needs the network.
    std::string sandboxError;
    if (!denyNetwork(sandboxError))
        log("network not blocked: " + sandboxError);
    const std::string modelDir = argv[1];
    const std::string vadModel = std::string(PARROTFISH_VOICE_DATA) + "/silero_vad.onnx";

    // Only the big cores: the little ones slow the decoding down.
    std::vector<int> big = bigCores();
    int threads = big.empty() ? 2 : static_cast<int>(big.size());
    if (const char *t = std::getenv("PARROTFISH_VOICE_THREADS"))
        threads = std::max(1, std::atoi(t));
    threads = std::min(threads, 4);
    if (!big.empty()) {
        cpu_set_t set;
        CPU_ZERO(&set);
        for (int c : big)
            CPU_SET(c, &set);
        sched_setaffinity(0, sizeof(set), &set);
    }

    Recognizer recognizer;
    Worker worker(recognizer);
    Commands commands;

    emit("loading");
    std::thread loader([&] {
        const double t0 = now();
        if (recognizer.load(modelDir, threads)) {
            log("model loaded in " + std::to_string(now() - t0) + " s, " + std::to_string(threads) + " threads");
            worker.setReady();
            emit("ready");
        } else {
            emit("error model");
        }
    });
    loader.detach();

    int session = 0;
    double idleSince = now();
    std::vector<float> lastSpeech;   // the end of the previous recording's speech
    for (;;) {
        const std::string command = commands.next(1.0);
        if (command == "quit")
            break;
        if (command != "start" && command != "continue") {
            if (command.empty() && now() - idleSince > IdleExitSeconds)
                break;
            continue;
        }

        const int id = ++session;
        const auto previous = command == "continue" && !lastSpeech.empty()
                                  ? std::make_shared<std::vector<float>>(lastSpeech) : nullptr;
        auto audio = std::make_unique<Audio>();
        Vad vad;
        std::string error;
        if (!audio->open(error)) {
            emit("error microphone " + error);
            emit("idle");
            idleSince = now();
            continue;
        }
        if (!vad.open(vadModel)) {
            emit("error vad");
            emit("idle");
            idleSince = now();
            continue;
        }
        emit("listening");

        std::vector<float> recording;
        recording.reserve(Rate * 30);
        auto live = std::make_shared<LiveText>();
        Phrases phrases(previous != nullptr);

        auto window = [&](int64_t from, int64_t to) {
            from = std::max<int64_t>(0, from);
            to = std::min<int64_t>(recording.size(), to);
            return std::vector<float>(recording.begin() + from, recording.begin() + std::max(from, to));
        };
        auto decode = [&](const Segment &segment) {
            const Plan plan = phrases.add(segment);
            auto samples = window(plan.from, plan.to);
            float left = 0;
            if (plan.kind == Plan::AfterPrevious) {
                left = float(previous->size()) / Rate;
                samples.insert(samples.begin(), previous->begin(), previous->end());
            }
            worker.add(id, [samples, plan, left, live, id, &worker](Recognizer &r) {
                const double t0 = now();
                const std::string fixed = live->decode(r, plan, samples, left);
                Recognizer::logTime(t0, samples.size());
                if (!worker.dropped(id)) {
                    if (plan.kind != Plan::AfterPrevious)
                        emit(phraseLine("fixed", plan.phrase - 1, fixed));
                    emit(phraseLine("open", plan.phrase, live->open));
                }
            });
        };

        bool cancelled = false;
        std::vector<float> chunk(Window);
        for (;;) {
            if (commands.pending()) {
                const std::string c = commands.next(0);
                if (c == "stop")
                    break;
                if (c == "quit")
                    std::exit(0);
                if (c == "cancel") {
                    cancelled = true;
                    break;
                }
            }
            if (!audio->read(chunk, error)) {
                emit("error microphone " + error);
                break;
            }
            recording.insert(recording.end(), chunk.begin(), chunk.end());
            for (const Segment &segment : vad.feed(chunk.data()))
                decode(segment);
            float sum = 0.0f;
            for (float v : chunk)
                sum += v * v;
            const float rms = std::sqrt(sum / Window);
            const float level = std::max(0.0f, std::min(1.0f, (20.0f * std::log10(rms + 1e-6f) + 60.0f) / 50.0f));
            char buf[32];
            std::snprintf(buf, sizeof(buf), "level %.2f", level);
            emit(buf);
            if (recording.size() >= static_cast<size_t>(MaxSeconds * Rate))
                break;
        }
        audio.reset();

        if (cancelled) {
            worker.drop(id);
            emit("idle");
        } else {
            for (const Segment &segment : vad.finish())
                decode(segment);
            if (!phrases.empty())
                lastSpeech = window(phrases.end() - ContextSeconds * Rate, phrases.end());
            worker.add(id, [live, id, &worker](Recognizer &) {
                if (!worker.dropped(id))
                    emit("final " + live->all());
                emit("idle");
            });
        }
        idleSince = now();
    }
    return 0;
}
