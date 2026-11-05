// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later

// Language tracking on mixed texts.
//
//   lang-bench -d build/data/en.pfd -d build/data/it.pfd -d build/data/de.pfd
//              -t tests/data [-n 300] [-s SEED] [-P name=value]...
//
// Texts are built from real sentences (tests/data/sentences-<lang>.txt)
// for every ordered pair of languages A, B:
//   single  A A + one word of B + A      B must not take the lead
//   phrase  A A + three words of B + A   informational
//   switch  A A + B B                    how many words until B leads
// "Confident" means the leading language reaches autocorrectConfidence,
// so autocorrect would act in it.

#include "parrotfish/engine.h"
#include "parrotfish/utf8.h"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <map>
#include <random>

using namespace parrotfish;

namespace {

using Sentence = std::vector<std::u32string>;

std::vector<Sentence> load(const std::string &path)
{
    std::vector<Sentence> out;
    std::ifstream in(path);
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#')
            continue;
        Sentence s = words(utf8::decode(line));
        if (s.size() >= 4)
            out.push_back(s);
    }
    return out;
}

struct Totals {
    int texts = 0;
    // single word / phrase
    int tookLead = 0;            // B became the leading language
    int confidentWrong = 0;      // B led with autocorrect confidence
    double pausedAfter = 0;      // A words after the insertion until A is confident again
    // switch
    double leadLatency = 0;      // words of B until B leads (and keeps leading)
    double confidentLatency = 0; // words of B until B is confident
    int neverSwitched = 0;
    double wrongConfident = 0;   // B words during which A was still confidently leading
    // monolingual start
    double startLatency = 0;     // words until A is confident at the start
};

} // namespace

int main(int argc, char **argv)
{
    std::vector<std::string> dictPaths;
    std::string dataDir = "tests/data";
    int n = 300;
    unsigned seed = 1;
    bool verbose = false;
    EngineParams params;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "-d" && i + 1 < argc) dictPaths.push_back(argv[++i]);
        else if (a == "-t" && i + 1 < argc) dataDir = argv[++i];
        else if (a == "-n" && i + 1 < argc) n = std::stoi(argv[++i]);
        else if (a == "-s" && i + 1 < argc) seed = std::stoul(argv[++i]);
        else if (a == "-v") verbose = true;
        else if (a == "-P" && i + 1 < argc) {
            if (!setParam(params, argv[++i])) {
                std::fprintf(stderr, "unknown parameter %s\n", argv[i]);
                return 2;
            }
        }
    }
    Engine engine(params);
    std::vector<std::vector<Sentence>> sentences;
    for (const std::string &p : dictPaths) {
        std::string error;
        if (!engine.addLanguage(p, &error)) {
            std::fprintf(stderr, "%s\n", error.c_str());
            return 1;
        }
        sentences.push_back(load(dataDir + "/sentences-" + engine.language(sentences.size()) + ".txt"));
        if (sentences.back().empty()) {
            std::fprintf(stderr, "no sentences for %s in %s\n",
                         engine.language(sentences.size() - 1).c_str(), dataDir.c_str());
            return 1;
        }
    }
    const size_t L = engine.languageCount();
    if (L < 2) {
        std::fprintf(stderr, "usage: lang-bench -d DICT -d DICT... [-t DIR] [-n N] [-P name=value]...\n");
        return 2;
    }
    const double conf = params.autocorrectConfidence;
    std::vector<const Dictionary *> dicts;
    for (size_t i = 0; i < L; ++i)
        dicts.push_back(&engine.dictionary(i));
    LanguageTracker tracker(dicts, params.language);
    std::mt19937 rng(seed);
    auto pick = [&](size_t lang) -> const Sentence & {
        return sentences[lang][std::uniform_int_distribution<size_t>(0, sentences[lang].size() - 1)(rng)];
    };
    auto confidentIn = [&](size_t lang) {
        return tracker.leading() == static_cast<int>(lang) && tracker.confidence() >= conf;
    };

    Totals single, phrase, sw, mono;
    for (size_t a = 0; a < L; ++a) {
        for (size_t b = 0; b < L; ++b) {
            if (a == b)
                continue;
            for (int t = 0; t < n; ++t) {
                // Start: two sentences of A.
                tracker.reset();
                int started = -1, count = 0;
                for (int k = 0; k < 2; ++k)
                    for (const auto &w : pick(a)) {
                        tracker.observe(w);
                        ++count;
                        if (started < 0 && confidentIn(a))
                            started = count;
                    }
                ++mono.texts;
                mono.startLatency += started < 0 ? count : started;

                // Insertions: one word, or three words, of B, then A again.
                for (int kind = 0; kind < 2; ++kind) {
                    Totals &tot = kind == 0 ? single : phrase;
                    const LanguageTracker saved = tracker;
                    const Sentence &bs = pick(b);
                    const size_t len = kind == 0 ? 1 : 3;
                    std::vector<std::u32string> ins;
                    if (kind == 0) {
                        // A word of at least 3 letters, as single foreign words usually are.
                        std::vector<std::u32string> longer;
                        for (const auto &w : bs)
                            if (w.size() >= 3) longer.push_back(w);
                        if (longer.empty()) continue;
                        ins.push_back(longer[std::uniform_int_distribution<size_t>(0, longer.size() - 1)(rng)]);
                    } else {
                        if (bs.size() < len) continue;
                        size_t at = std::uniform_int_distribution<size_t>(0, bs.size() - len)(rng);
                        ins.assign(bs.begin() + at, bs.begin() + at + len);
                    }
                    bool lead = false, confWrong = false;
                    for (const auto &w : ins) {
                        tracker.observe(w);
                        lead |= tracker.leading() == static_cast<int>(b);
                        confWrong |= confidentIn(b);
                    }
                    int paused = 0;
                    bool recovered = false;
                    std::string trace;
                    auto note = [&](const std::u32string &w) {
                        if (!verbose) return;
                        trace += " " + utf8::encode(w) + "[";
                        for (double x : tracker.weights()) trace += std::to_string(int(x * 100)) + " ";
                        trace.back() = ']';
                    };
                    for (const auto &w : ins) note(w);
                    for (const auto &w : pick(a)) {
                        tracker.observe(w);
                        note(w);
                        lead |= tracker.leading() == static_cast<int>(b);
                        confWrong |= confidentIn(b);
                        if (!recovered && confidentIn(a)) recovered = true;
                        if (!recovered) ++paused;
                    }
                    if (verbose && kind == 0 && lead)
                        std::printf("%s→%s:%s\n", engine.language(a).c_str(), engine.language(b).c_str(), trace.c_str());
                    ++tot.texts;
                    tot.tookLead += lead;
                    tot.confidentWrong += confWrong;
                    tot.pausedAfter += paused;
                    tracker = saved;   // back to the state before the insertion
                }

                // Real switch: two sentences of B.
                int leadAt = -1, confAt = -1, wrongConf = 0, k = 0;
                for (int s = 0; s < 2; ++s)
                    for (const auto &w : pick(b)) {
                        tracker.observe(w);
                        ++k;
                        const bool leads = tracker.leading() == static_cast<int>(b);
                        if (leads && leadAt < 0) leadAt = k;
                        if (!leads) leadAt = -1;   // must keep the lead
                        if (confAt < 0 && confidentIn(b)) confAt = k;
                        if (confidentIn(a)) ++wrongConf;
                    }
                ++sw.texts;
                if (leadAt < 0) { ++sw.neverSwitched; leadAt = k; }
                sw.leadLatency += leadAt;
                sw.confidentLatency += confAt < 0 ? k : confAt;
                sw.wrongConfident += wrongConf;
            }
        }
    }

    std::printf("languages:");
    for (size_t i = 0; i < L; ++i) std::printf(" %s", engine.language(i).c_str());
    const LanguageParams &lp = params.language;
    std::printf("  (switchProb %.3g maxEvidence %.3g shortWordWeight %.3g unknownFreq %d"
                " autocorrectConfidence %.2g)\n",
                lp.switchProb, lp.maxEvidence, lp.shortWordWeight, lp.unknownFreq, conf);
    std::printf("start:   %.2f words until confident\n", mono.startLatency / mono.texts);
    std::printf("single:  took lead %.1f%%, confident wrong %.1f%%, paused %.2f words after\n",
                100.0 * single.tookLead / single.texts, 100.0 * single.confidentWrong / single.texts,
                single.pausedAfter / single.texts);
    std::printf("phrase:  took lead %.1f%%, confident wrong %.1f%%, paused %.2f words after\n",
                100.0 * phrase.tookLead / phrase.texts, 100.0 * phrase.confidentWrong / phrase.texts,
                phrase.pausedAfter / phrase.texts);
    std::printf("switch:  lead after %.2f words, confident after %.2f, old language confident"
                " for %.2f words, never %.1f%%\n",
                sw.leadLatency / sw.texts, sw.confidentLatency / sw.texts,
                sw.wrongConfident / sw.texts, 100.0 * sw.neverSwitched / sw.texts);
    return 0;
}
