// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD

// Correction and autocorrect in running text (milestone 1/MVP).
//
//   sentence-bench -l data/layouts/it.layout -t tests/data/tatoeba-it.txt
//                  -d build/data/it.pfd [-d OTHER.pfd]... [-n 2000] [-r 0.3]
//                  [-s SEED] [-m LANG] [-B] [-F] [-D] [-P name=value]... [-v]
//
// Words of real sentences are typed one by one with the earlier words as
// context, as on the phone (all dictionaries loaded, equal starting weights,
// completions on). A share -r of the words gets one typo (the kinds of
// typos.h); the rest are typed correctly and must not be changed.
//
// Each sentence is a new text. -m LANG starts it leaning towards LANG, as
// the plugin does for the language the last text was in (×3); pass the
// sentences' language or another one (the user switched languages). -B adds
// the results for the first word of a text, for other words of two typed
// letters and for the rest (with -v: each word's group and confidence); -F
// types only the first words (with the same typos). -D adds two kinds of
// typos: one letter of a double missed, a letter tapped twice (other typos
// change too).

#include "parrotfish/engine.h"
#include "typos.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <sstream>

using namespace parrotfish;
using namespace bench;

namespace {

struct Stats {
    int total = 0, realWord = 0, top1 = 0, top3 = 0, fixed = 0, wrong = 0;
};

// Where a word stands, for -B.
enum Group { First, Short, Rest, GroupCount };
const char *const groupNames[] = {"word 1", "2 letters", "rest"};

struct GroupStats {
    int clean = 0, cleanWrong = 0, fixable = 0, fixed = 0, wrong = 0;
};

double pct(int a, int b)
{
    return b ? 100.0 * a / b : 0.0;
}

} // namespace

int main(int argc, char **argv)
{
    std::string layoutPath, textPath;
    std::vector<std::string> dictPaths;
    int maxSentences = 2000;
    double rate = 0.3;
    unsigned seed = 1;
    bool verbose = false, breakdown = false, firstOnly = false, doubles = false;
    std::string remembered;
    EngineParams params;
    params.corrector.completions = true;   // as on the phone
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "-l" && i + 1 < argc) layoutPath = argv[++i];
        else if (a == "-t" && i + 1 < argc) textPath = argv[++i];
        else if (a == "-d" && i + 1 < argc) dictPaths.push_back(argv[++i]);
        else if (a == "-n" && i + 1 < argc) maxSentences = std::stoi(argv[++i]);
        else if (a == "-r" && i + 1 < argc) rate = std::stod(argv[++i]);
        else if (a == "-s" && i + 1 < argc) seed = std::stoul(argv[++i]);
        else if (a == "-m" && i + 1 < argc) remembered = argv[++i];
        else if (a == "-B") breakdown = true;
        else if (a == "-F") firstOnly = breakdown = true;
        else if (a == "-D") doubles = true;
        else if (a == "-v") verbose = true;
        else if (a == "-P" && i + 1 < argc) {
            if (!setParam(params, argv[++i])) {
                std::fprintf(stderr, "unknown parameter %s\n", argv[i]);
                return 2;
            }
        }
    }
    std::ifstream lf(layoutPath);
    std::stringstream ls;
    ls << lf.rdbuf();
    const KeyModel keys = KeyModel::parse(ls.str());
    Engine engine(params);
    for (const std::string &p : dictPaths) {
        std::string error;
        if (!engine.addLanguage(p, &error)) {
            std::fprintf(stderr, "%s\n", error.c_str());
            return 1;
        }
    }
    std::ifstream tf(textPath);
    if (keys.keys().empty() || engine.languageCount() == 0 || !tf) {
        std::fprintf(stderr, "usage: sentence-bench -l LAYOUT -t SENTENCES -d DICT [-d DICT]... [-n N] [-r RATE]"
                             " [-s SEED] [-m LANG] [-B] [-F] [-D] [-P name=value]... [-v]\n");
        return 2;
    }
    std::vector<double> prior;
    if (!remembered.empty()) {
        const int k = engine.languageIndex(remembered);
        if (k < 0) {
            std::fprintf(stderr, "-m %s: no such language loaded\n", remembered.c_str());
            return 2;
        }
        prior.assign(engine.languageCount(), 1.0);
        prior[k] *= 3.0;   // as PredictionEngine::setContext
    }
    engine.setLayout(keys);
    Typos typos(keys, engine.dictionary(0), seed);
    std::uniform_real_distribution<double> coin(0.0, 1.0);

    Stats stats[KindCount];
    GroupStats groups[GroupCount];
    double totalUs = 0, maxUs = 0;
    int searches = 0, sentences = 0;
    std::string line;
    while (std::getline(tf, line) && sentences < maxSentences) {
        if (line.empty() || line[0] == '#')
            continue;
        ++sentences;
        const std::vector<std::u32string> ws = words(utf8::decode(line));
        std::u32string context;
        for (const std::u32string &target : ws) {
            const bool plain = target.size() >= 2 && std::all_of(target.begin(), target.end(),
                [](char32_t c) { return isLetter(c) || c == U'\''; });
            if (plain) {
                Kind kind = Clean;
                std::u32string typed = target;
                if (coin(typos.rng()) < rate) {
                    kind = Kind(Neighbour + typos.pos((doubles ? DoubleExtra : Double) - Neighbour + 1));
                    if (kind == Accent && !typos.hasAccents(target))
                        kind = Neighbour;
                    typed = typos.apply(target, kind);
                    // Keep the capital of a sentence start, as typed with Shift.
                    if (!typed.empty() && utf8::isUpper(target[0]))
                        typed[0] = utf8::toUpper(typed[0]);
                    if (typed.size() < 2 || lower(typed) == lower(target)) {
                        kind = Clean;
                        typed = target;
                    }
                }
                if (firstOnly && !context.empty()) {
                    context += target + U" ";
                    continue;   // the same typos as without -F
                }
                engine.startText(context, prior);
                const auto t0 = std::chrono::steady_clock::now();
                const Result r = engine.process(tapsFor(typed));
                const double us = std::chrono::duration<double, std::micro>(
                    std::chrono::steady_clock::now() - t0).count();
                totalUs += us;
                maxUs = std::max(maxUs, us);
                ++searches;

                Stats &s = stats[kind];
                ++s.total;
                const bool fired = r.autocorrect >= 0;
                const std::u32string got = fired ? r.suggestions[r.autocorrect].word : typed;
                const int group = context.empty() ? First : typed.size() <= 2 ? Short : Rest;
                GroupStats &g = groups[group];
                char tag[48] = "";
                if (breakdown)
                    std::snprintf(tag, sizeof tag, " [%s, %.2f]", groupNames[group], r.confidence);
                if (kind == Clean) {
                    ++g.clean;
                } else if (!r.typedIsWord) {
                    ++g.fixable;
                    if (fired && lower(got) == lower(target)) ++g.fixed;
                    if (fired && lower(got) != lower(target)) ++g.wrong;
                }
                if (kind == Clean) {
                    if (lower(got) != lower(target)) {
                        ++s.wrong;
                        ++g.cleanWrong;
                        if (verbose)
                            std::printf("changed  %s → %s%s\n", utf8::encode(typed).c_str(), utf8::encode(got).c_str(), tag);
                    }
                } else if (r.typedIsWord) {
                    ++s.realWord;
                } else {
                    int rank = -1;
                    for (size_t k = 0; k < r.suggestions.size() && k < 3; ++k)
                        if (lower(r.suggestions[k].word) == lower(target)) { rank = static_cast<int>(k); break; }
                    if (rank == 0) ++s.top1;
                    if (rank >= 0) ++s.top3;
                    if (fired && lower(got) == lower(target)) ++s.fixed;
                    if (fired && lower(got) != lower(target)) ++s.wrong;
                    if (verbose && (rank != 0 || !fired || lower(got) != lower(target)))
                        std::printf("%-13s %-14s → %-14s got %-14s rank %d%s%s\n", kindName(kind),
                                    utf8::encode(typed).c_str(), utf8::encode(target).c_str(),
                                    r.suggestions.empty() ? "-" : utf8::encode(r.suggestions[0].word).c_str(),
                                    rank, fired ? " (autocorrected)" : "", tag);
                }
            }
            context += target + U" ";
        }
    }

    std::printf("%s, %zu languages, %d sentences, typo rate %.2f, seed %u\n", textPath.c_str(),
                engine.languageCount(), sentences, rate, seed);
    std::printf("%-14s %6s %8s %7s %7s %10s %10s\n", "typo", "words", "realword", "top1", "top3",
                "autofixed", "autowrong");
    Stats all;
    for (int k = 0; k < KindCount; ++k) {
        const Stats &s = stats[k];
        if (!s.total) continue;
        if (k == Clean) {
            std::printf("%-14s %6d %8s %7s %7s %10s %9.2f%%\n", kindName(k), s.total, "", "", "", "",
                        pct(s.wrong, s.total));
            continue;
        }
        const int fixable = s.total - s.realWord;
        std::printf("%-14s %6d %7.1f%% %6.1f%% %6.1f%% %9.1f%% %9.1f%%\n", kindName(k), s.total,
                    pct(s.realWord, s.total), pct(s.top1, fixable), pct(s.top3, fixable),
                    pct(s.fixed, fixable), pct(s.wrong, fixable));
        all.total += s.total; all.realWord += s.realWord; all.top1 += s.top1; all.top3 += s.top3;
        all.fixed += s.fixed; all.wrong += s.wrong;
    }
    const int fixable = all.total - all.realWord;
    std::printf("%-14s %6d %7.1f%% %6.1f%% %6.1f%% %9.1f%% %9.1f%%\n", "all typos", all.total,
                pct(all.realWord, all.total), pct(all.top1, fixable), pct(all.top3, fixable),
                pct(all.fixed, fixable), pct(all.wrong, fixable));
    if (breakdown) {
        std::printf("start lean: %s\n", remembered.empty() ? "none" : remembered.c_str());
        std::printf("%-10s %6s %12s %5s %12s %12s\n", "words", "clean", "changed", "typos",
                    "autofixed", "autowrong");
        for (int k = 0; k < GroupCount; ++k) {
            const GroupStats &g = groups[k];
            std::printf("%-10s %6d %5d %5.2f%% %5d %5d %5.1f%% %5d %5.1f%%\n", groupNames[k], g.clean,
                        g.cleanWrong, pct(g.cleanWrong, g.clean), g.fixable, g.fixed,
                        pct(g.fixed, g.fixable), g.wrong, pct(g.wrong, g.fixable));
        }
    }
    std::printf("time per word: mean %.0f µs, max %.0f µs\n", totalUs / searches, maxUs);
    return 0;
}
